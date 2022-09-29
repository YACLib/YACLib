#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <cstddef>

#include <gtest/gtest.h>

namespace test {
namespace {

class Core : public yaclib::IRef {
 public:
  static std::size_t sInstances;

  Core(const Core&) = delete;
  Core& operator=(const Core&) = delete;

  Core() {
    ++sInstances;
  }

  ~Core() override {
    --sInstances;
  }
};

std::size_t Core::sInstances = 0;

class X : public Core {};
class Y : public X {};

using CounterX = yaclib::detail::Helper<yaclib::detail::AtomicCounter, X>;
using CounterY = yaclib::detail::Helper<yaclib::detail::AtomicCounter, Y>;

template <typename T>
auto MakeIntrusive() {
  return yaclib::MakeShared<T>(1);
}

TEST(ctor, default) {
  yaclib::IntrusivePtr<Core> px;
  EXPECT_EQ(px.Get(), nullptr);
  EXPECT_EQ(px, nullptr);
  EXPECT_EQ(nullptr, px);
}

TEST(ctor, pointer) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<Core> px{nullptr};
    EXPECT_EQ(px, nullptr);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<Core> px{yaclib::NoRefTag{}, nullptr};
    EXPECT_EQ(px, nullptr);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    auto* p = new CounterX{1};
    EXPECT_EQ(p->GetRef(), 1);

    EXPECT_EQ(Core::sInstances, 1);

    yaclib::IntrusivePtr<Core> px{yaclib::NoRefTag{}, p};
    EXPECT_EQ(px.Get(), p);
    EXPECT_EQ(px, p);
    EXPECT_EQ(p, px);
    EXPECT_EQ(p->GetRef(), 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    auto* p = new CounterX{1};
    EXPECT_EQ(p->GetRef(), 1);

    p->IncRef();
    EXPECT_EQ(p->GetRef(), 2);

    yaclib::IntrusivePtr<Core> pb{yaclib::NoRefTag{}, p};
    {
      yaclib::IntrusivePtr<CounterX> pc{yaclib::NoRefTag{}, p};
      EXPECT_EQ(pb, pc);
      EXPECT_EQ(p->GetRef(), 2);
    }
    EXPECT_EQ(p->GetRef(), 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

TEST(ctor, copy) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<CounterX> pc1;
    yaclib::IntrusivePtr<CounterX> pc2{pc1};
    EXPECT_EQ(pc1, pc2);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<Y> py;
    yaclib::IntrusivePtr<X> px{py};
    EXPECT_EQ(py, px);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<X> px1 = MakeIntrusive<X>();
    yaclib::IntrusivePtr<X> px2(px1);
    EXPECT_EQ(px1, px2);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<Y> py = MakeIntrusive<Y>();
    yaclib::IntrusivePtr<X> px(py);
    EXPECT_EQ(py, px);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

TEST(dtor, simple) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<CounterX> pc1;
    yaclib::IntrusivePtr<CounterX> pc2{pc1};
    EXPECT_EQ(pc1, pc2);
  }
  EXPECT_EQ(Core::sInstances, 0);

  {
    auto* x = new CounterX{1};
    yaclib::IntrusivePtr<X> px1{yaclib::NoRefTag{}, x};
    EXPECT_EQ(x->GetRef(), 1);
    EXPECT_EQ(Core::sInstances, 1);
    {
      yaclib::IntrusivePtr<X> px2{x};
      EXPECT_EQ(x->GetRef(), 2);
      EXPECT_EQ(Core::sInstances, 1);
    }
    EXPECT_EQ(x->GetRef(), 1);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

TEST(assign, copy) {
  EXPECT_EQ(Core::sInstances, 0);

  {
    yaclib::IntrusivePtr<X> p1;
    p1.operator=(p1);
    EXPECT_EQ(p1, p1);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    yaclib::IntrusivePtr<X> p2;

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    yaclib::IntrusivePtr<X> p3(p1);

    p1 = p3;

    EXPECT_EQ(p1, p3);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    yaclib::IntrusivePtr<X> p4 = MakeIntrusive<X>();

    EXPECT_EQ(Core::sInstances, 1);

    p1 = p4;

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterX&>(*p1).GetRef(), 2);

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterX&>(*p4).GetRef(), 1);

    p4 = p3;

    EXPECT_EQ(p4, p3);
    EXPECT_EQ(Core::sInstances, 0);
  }
}

TEST(assign, conversation) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<X> p1;

    yaclib::IntrusivePtr<Y> p2;

    p1 = p2;
    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    yaclib::IntrusivePtr<Y> p4 = MakeIntrusive<Y>();

    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 1);

    yaclib::IntrusivePtr<X> p5(p4);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p1 = p4;

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterY&>(*p1).GetRef(), 3);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 3);

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p4 = p2;
    p5 = p2;

    EXPECT_EQ(p4, p2);
    EXPECT_EQ(Core::sInstances, 0);
  }
}

TEST(assign, pointer) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    yaclib::IntrusivePtr<X> p1;

    p1 = p1.Get();

    EXPECT_EQ(p1, p1);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    yaclib::IntrusivePtr<X> p2;

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    yaclib::IntrusivePtr<X> p3(p1);

    p1 = p3.Get();

    EXPECT_EQ(p1, p3);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    yaclib::IntrusivePtr<X> p4 = MakeIntrusive<X>();

    EXPECT_EQ(Core::sInstances, 1);

    p1 = p4.Get();

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterX&>(*p1).GetRef(), 2);

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);

    p4 = p3.Get();

    EXPECT_EQ(p4, p3);
    EXPECT_EQ(Core::sInstances, 0);
  }

  {
    yaclib::IntrusivePtr<X> p1;
    yaclib::IntrusivePtr<Y> p2;

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    yaclib::IntrusivePtr<Y> p4 = MakeIntrusive<Y>();

    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 1);

    yaclib::IntrusivePtr<X> p5(p4);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p1 = p4.Get();

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterY&>(*p1).GetRef(), 3);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 3);

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p4 = p2.Get();
    p5 = p2.Get();

    EXPECT_EQ(p4, p2);
    EXPECT_EQ(Core::sInstances, 0);
  }
}
//
// TEST(reset, simple) {
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset();
//    BOOST_TEST(px.get() == 0);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    px.reset();
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0, false);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0, true);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    yaclib::IntrusivePtr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset(p, true);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    intrusive_ptr_add_ref(p);
//    BOOST_TEST(p->use_count() == 1);
//
//    yaclib::IntrusivePtr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset(p, false);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p, true);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    intrusive_ptr_add_ref(p);
//    BOOST_TEST(p->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p, false);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//}
//
// TEST(access, simple) {
//  {
//    yaclib::IntrusivePtr<X> px;
//    BOOST_TEST(px ? false : true);
//    BOOST_TEST(!px);
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    yaclib::IntrusivePtr<X> px(0);
//    BOOST_TEST(px ? false : true);
//    BOOST_TEST(!px);
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    yaclib::IntrusivePtr<X> px(new X);
//    BOOST_TEST(px ? true : false);
//    BOOST_TEST(!!px);
//    BOOST_TEST(&*px == px.get());
//    BOOST_TEST(px.operator->() == px.get());
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    yaclib::IntrusivePtr<X> px;
//    X* detached = px.detach();
//    BOOST_TEST(px.get() == 0);
//    BOOST_TEST(detached == 0);
//  }
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    yaclib::IntrusivePtr<X> px(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    X* detached = px.detach();
//    BOOST_TEST(px.get() == 0);
//
//    BOOST_TEST(detached == p);
//    BOOST_TEST(detached->use_count() == 1);
//
//    delete detached;
//  }
//}

// TEST(swap, simple) {
//   {
//     yaclib::IntrusivePtr<X> px;
//     yaclib::IntrusivePtr<X> px2;
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == 0);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   {
//     X* p = new X;
//     yaclib::IntrusivePtr<X> px;
//     yaclib::IntrusivePtr<X> px2(p);
//     yaclib::IntrusivePtr<X> px3(px2);
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == p);
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(px2.get() == 0);
//     BOOST_TEST(px3.get() == p);
//     BOOST_TEST(px3->use_count() == 2);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == p);
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px3.get() == p);
//     BOOST_TEST(px3->use_count() == 2);
//   }
//
//   {
//     X* p1 = new X;
//     X* p2 = new X;
//     yaclib::IntrusivePtr<X> px(p1);
//     yaclib::IntrusivePtr<X> px2(p2);
//     yaclib::IntrusivePtr<X> px3(px2);
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == p2);
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(px2.get() == p1);
//     BOOST_TEST(px2->use_count() == 1);
//     BOOST_TEST(px3.get() == p2);
//     BOOST_TEST(px3->use_count() == 2);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == p1);
//     BOOST_TEST(px->use_count() == 1);
//     BOOST_TEST(px2.get() == p2);
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px3.get() == p2);
//     BOOST_TEST(px3->use_count() == 2);
//   }
// }
//
// namespace n_comparison {
//
// template <class T, class U>
// void test2(yaclib::IntrusivePtr<T> const& p, yaclib::IntrusivePtr<U> const&
// q) {
//   BOOST_TEST((p == q) == (p.get() == q.get()));
//   BOOST_TEST((p != q) == (p.get() != q.get()));
// }
//
// template <class T>
// void test3(yaclib::IntrusivePtr<T> const& p, yaclib::IntrusivePtr<T> const&
// q) {
//   BOOST_TEST((p == q) == (p.get() == q.get()));
//   BOOST_TEST((p.get() == q) == (p.get() == q.get()));
//   BOOST_TEST((p == q.get()) == (p.get() == q.get()));
//   BOOST_TEST((p != q) == (p.get() != q.get()));
//   BOOST_TEST((p.get() != q) == (p.get() != q.get()));
//   BOOST_TEST((p != q.get()) == (p.get() != q.get()));
//
//   // 'less' moved here as a g++ 2.9x parse error workaround
//   std::less<T*> less;
//   BOOST_TEST((p < q) == less(p.get(), q.get()));
// }
//
// void test() {
//   {
//     yaclib::IntrusivePtr<X> px;
//     test3(px, px);
//
//     yaclib::IntrusivePtr<X> px2;
//     test3(px, px2);
//
//     yaclib::IntrusivePtr<X> px3(px);
//     test3(px3, px3);
//     test3(px, px3);
//   }
//
//   {
//     yaclib::IntrusivePtr<X> px;
//
//     yaclib::IntrusivePtr<X> px2(new X);
//     test3(px, px2);
//     test3(px2, px2);
//
//     yaclib::IntrusivePtr<X> px3(new X);
//     test3(px2, px3);
//
//     yaclib::IntrusivePtr<X> px4(px2);
//     test3(px2, px4);
//     test3(px4, px4);
//   }
//
//   {
//     yaclib::IntrusivePtr<X> px(new X);
//
//     yaclib::IntrusivePtr<Y> py(new Y);
//     test2(px, py);
//
//     yaclib::IntrusivePtr<X> px2(py);
//     test2(px2, py);
//     test3(px, px2);
//     test3(px2, px2);
//   }
// }
//
// }  // namespace n_comparison
//
// namespace n_static_cast {
//
// void test() {
//   {
//     yaclib::IntrusivePtr<X> px(new Y);
//
//     yaclib::IntrusivePtr<Y> py = boost::static_pointer_cast<Y>(px);
//     BOOST_TEST(px.get() == py.get());
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(py->use_count() == 2);
//
//     yaclib::IntrusivePtr<X> px2(py);
//     BOOST_TEST(px2.get() == px.get());
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     yaclib::IntrusivePtr<Y> py =
//         boost::static_pointer_cast<Y>(yaclib::IntrusivePtr<X>(new Y));
//     BOOST_TEST(py.get() != 0);
//     BOOST_TEST(py->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_static_cast
//
// namespace n_const_cast {
//
// void test() {
//   {
//     yaclib::IntrusivePtr<X const> px;
//
//     yaclib::IntrusivePtr<X> px2 = boost::const_pointer_cast<X>(px);
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   {
//     yaclib::IntrusivePtr<X> px2 =
//         boost::const_pointer_cast<X>(yaclib::IntrusivePtr<X const>());
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     yaclib::IntrusivePtr<X const> px(new X);
//
//     yaclib::IntrusivePtr<X> px2 = boost::const_pointer_cast<X>(px);
//     BOOST_TEST(px2.get() == px.get());
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px->use_count() == 2);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     yaclib::IntrusivePtr<X> px =
//         boost::const_pointer_cast<X>(yaclib::IntrusivePtr<X const>(new X));
//     BOOST_TEST(px.get() != 0);
//     BOOST_TEST(px->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_const_cast
//
// namespace n_dynamic_cast {
//
// void test() {
//   {
//     yaclib::IntrusivePtr<X> px;
//
//     yaclib::IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     yaclib::IntrusivePtr<Y> py =
//         boost::dynamic_pointer_cast<Y>(yaclib::IntrusivePtr<X>());
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     yaclib::IntrusivePtr<X> px(static_cast<X*>(0));
//
//     yaclib::IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     yaclib::IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(
//         yaclib::IntrusivePtr<X>(static_cast<X*>(0)));
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     yaclib::IntrusivePtr<X> px(new X);
//
//     yaclib::IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     yaclib::IntrusivePtr<Y> py =
//         boost::dynamic_pointer_cast<Y>(yaclib::IntrusivePtr<X>(new X));
//     BOOST_TEST(py.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     yaclib::IntrusivePtr<X> px(new Y);
//
//     yaclib::IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == px.get());
//     BOOST_TEST(py->use_count() == 2);
//     BOOST_TEST(px->use_count() == 2);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     yaclib::IntrusivePtr<X> px(new Y);
//
//     yaclib::IntrusivePtr<Y> py =
//         boost::dynamic_pointer_cast<Y>(yaclib::IntrusivePtr<X>(new Y));
//     BOOST_TEST(py.get() != 0);
//     BOOST_TEST(py->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_dynamic_cast
//
// namespace n_transitive {
//
// struct X : public N::base {
//   yaclib::IntrusivePtr<X> next;
// };
//
// void test() {
//   yaclib::IntrusivePtr<X> p(new X);
//   p->next = yaclib::IntrusivePtr<X>(new X);
//   BOOST_TEST(!p->next->next);
//   p = p->next;
//   BOOST_TEST(!p->next);
// }
//
// }  // namespace n_transitive
//
// namespace n_report_1 {
//
// class foo : public N::base {
//  public:
//   foo() : m_self(this) {
//   }
//
//   void suicide() {
//     m_self = 0;
//   }
//
//  private:
//   yaclib::IntrusivePtr<foo> m_self;
// };
//
// void test() {
//   foo* foo_ptr = new foo;
//   foo_ptr->suicide();
// }
//
// }  // namespace n_report_1

}  // namespace
}  // namespace test
